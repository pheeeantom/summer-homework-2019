CREATE OR REPLACE FUNCTION validate() RETURNS TRIGGER AS $$
begin
	if (TG_OP = 'UPDATE') then
		if (NEW.ФИО <> OLD.ФИО) then
			if (NEW.ФИО !~ '^[А-Яа-я]+ [А-Яа-я]+ [А-Яа-я]+$') then
				raise exception 'Wrong name %', NEW.ФИО;
				RETURN null;
			end if;
		elsif (NEW.Паспорт <> OLD.Паспорт) then
			if (NEW.Паспорт !~ '^[0-9]{4}[ ]?[0-9]{6}$') then
				raise exception 'Wrong passport %', NEW.Паспорт;
				RETURN null;
			end if;
		elsif (NEW.Номер <> OLD.Номер) then
			if (NEW.Номер > 100 or NEW.Номер < 1) then
				raise exception 'Too big number %', NEW.Номер;
				RETURN null;
			end if;
		elsif (NEW."Дата заезда" <> OLD."Дата заезда") then
			if (NEW."Дата заезда" > OLD."Дата отъезда") then
				raise exception 'Wrong arrive date %', NEW."Дата заезда";
				RETURN null;
			end if;
		elsif (NEW."Дата отъезда" <> OLD."Дата отъезда") then
			if (NEW."Дата отъезда" < OLD."Дата заезда") then
				raise exception 'Wrong departure date %', NEW."Дата отъезда";
				RETURN null;
			end if;
		end if;
	elsif (TG_OP = 'INSERT') then
		if (NEW.ФИО !~ '^[А-Яа-я]+ [А-Яа-я]+ [А-Яа-я]+$') then
			raise exception 'Wrong name %', NEW.ФИО;
			RETURN null;
		end if;
		if (NEW.Паспорт !~ '^[0-9]{4}[ ]?[0-9]{6}$') then
			raise exception 'Wrong passport %', NEW.Паспорт;
			RETURN null;
		end if;
		if (NEW.Номер > 100 or NEW.Номер < 1) then
			raise exception 'Too big number %', NEW.Номер;
			RETURN null;
		end if;
		if (NEW."Дата отъезда" < NEW."Дата заезда") then
			raise exception 'Wrong dates % %', NEW."Дата заезда", NEW."Дата отъезда";
			RETURN null;
		end if;
	end if;
	return NEW;
end;
$$ LANGUAGE plpgsql;